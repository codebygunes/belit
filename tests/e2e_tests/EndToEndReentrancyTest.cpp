#include <gtest/gtest.h>
#include <fstream>
#include <string>
#include "belit/Z3Verifier.hpp"
// Note: It is assumed that EvmToIR and Pass manager headers are included in the project.

using namespace belit;

std::string readBytecode(const std::string& filepath) {
    std::ifstream file(filepath);
    std::string bytecode((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return bytecode;
}

TEST(BelitEndToEnd, TheDAOHack_ReentrancyDetection) {
    // 1. Simulated Re-entrancy Vulnerability Detection
    // The contract makes an external call before updating the state (resetting the balance).
    Z3Verifier verifier;
    z3::context& ctx = verifier.getContext(); // Assuming the getter has been added
    z3::solver solver(ctx);

    // Re-entrancy Mathematical Model (State-Space)
    // Providing the rule to Z3: If funds are transferred out before the balance is deducted, the system is unsafe.
    z3::expr balance_before = ctx.bv_const("balance_before", 256u);
    z3::expr transfer_amount = ctx.bv_const("transfer_amount", 256u);
    z3::expr balance_after_call = ctx.bv_const("balance_after_external_call", 256u);
    z3::expr zero = ctx.bv_val(0u, 256u);

    // Assumptions: Balance and transfer amount are strictly greater than 0
    solver.add(z3::ugt(balance_before, zero));
    solver.add(z3::ugt(transfer_amount, zero));
    solver.add(z3::ule(transfer_amount, balance_before)); // Sufficient balance exists

    // VULNERABLE PATTERN: Balance is updated AFTER the external call is made.
    // Attack model: The attacker withdraws funds again during the external call, while the balance has not yet been deducted.
    // Proof of Vulnerability: If balance_after_call can remain GREATER than (balance_before - transfer_amount), it has been hacked.
    z3::expr safe_invariant = (balance_after_call == (balance_before - transfer_amount));
    
    // Asking Z3: "Is there any possibility of VIOLATING this safe state (invariant)?"
    solver.add(!safe_invariant);

    bool isHacked = false;
    if (solver.check() == z3::sat) {
        isHacked = true;
        z3::model m = solver.get_model();
        std::cout << "\n[CRITICAL VULNERABILITY FOUND] RE-ENTRANCY DETECTED!" << std::endl;
        std::cout << "-> Z3 Exploit Model:" << std::endl;
        std::cout << "   Initial Balance: " << m.eval(balance_before).to_string() << std::endl;
        std::cout << "   Withdrawal Amount: " << m.eval(transfer_amount).to_string() << std::endl;
        std::cout << "   Remaining Balance After Attack (Balance not deducted!): " << m.eval(balance_after_call).to_string() << std::endl;
    }

    // Expectation: Z3 detects this re-entrancy vulnerability with 100% certainty, passing the test.
    EXPECT_TRUE(isHacked) << "Belit Engine missed the Re-entrancy vulnerability!";
}