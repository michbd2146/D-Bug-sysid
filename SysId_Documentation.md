# D-Bug SysId: System Identification and Control Theory Guide

Welcome to the D-Bug SysId documentation. This guide explains how to use the tool, the mathematical control theory driving it, and how to apply the generated data to real-world FRC mechanisms.

---

## 1. Purpose and Real-World Applications

The primary purpose of **D-Bug SysId** is to characterize the physical behavior of robot mechanisms (like elevators, arms, and drivetrains) and calculate optimal control gains for them. 

Without SysId, programmers often rely on "guess-and-check" tuning for PID loops, which can lead to oscillations, instability, or sluggish response. By recording telemetry data from the robot and feeding it into SysId, you can mathematically model the physical system. 

### Real-World Applications:
*   **Elevators:** Calculate the exact voltage needed to counteract gravity, overcoming static friction, and smoothly move to a setpoint without overshooting.
*   **Arms (Rotational Joints):** Account for the changing force of gravity as an arm extends horizontally versus vertically.
*   **Drivetrains (Tank/Swerve):** Ensure that both sides of the robot perfectly track velocity profiles during autonomous routines, minimizing drift and path deviation.

---

## 2. Mathematical Explanations and Control Logic

SysId uses system identification to derive Feedforward constants and uses Optimal Control theory (LQR) to generate Feedback (PID) constants.

### Feedforward Constants ($K_s, K_g, K_v, K_a$)
Feedforward control predicts the necessary voltage (or current) to achieve a desired state based on the physical model of the mechanism.
*   **$K_s$ (Static Friction):** The minimum voltage required to break static friction and cause the mechanism to begin moving.
*   **$K_g$ (Gravity):** The voltage required to exactly counteract gravity and hold an elevator or arm perfectly still in mid-air.
*   **$K_v$ (Velocity):** The voltage required to maintain a constant velocity of $1 \text{ m/s}$ (or rad/s).
*   **$K_a$ (Acceleration):** The voltage required to induce an acceleration of $1 \text{ m/s}^2$ (or rad/s²).

**The Base Physical Model:**
$$ V(t) = K_s \cdot \text{sgn}(\dot{x}) + K_g + K_v \cdot \dot{x} + K_a \cdot \ddot{x} $$

### State-Space Modeling
D-Bug SysId automatically generates State-Space matrices representing the physical system. State-Space is a powerful modern control technique that represents a system as a set of first-order differential equations:
$$ \dot{\mathbf{x}} = A\mathbf{x} + B\mathbf{u} $$
$$ \mathbf{y} = C\mathbf{x} + D\mathbf{u} $$
Where:
*   $A$ is the system matrix (how the current state affects the rate of change).
*   $B$ is the input matrix (how voltage affects the rate of change).
*   SysId also provides Discrete-Time matrices ($A_d, B_d$) calculated for your specific control loop period (e.g., $1\text{ms}$ for CTRE Phoenix 6).

### Linear-Quadratic Regulator (LQR) Tuning
In the **Feedback Analysis** section, SysId uses LQR to compute the optimal $K_p$ and $K_d$ gains. LQR balances a cost function $J$ based on two user inputs:
*   **$Q$ (State Cost):** How strictly do you want to eliminate position/velocity errors? (Input as Max Position/Velocity Error).
*   **$R$ (Effort Cost):** How much voltage/current are you willing to spend to fix the error? (Input as Max Control Effort).

LQR mathematically guarantees the most efficient PID response to achieve your specified tolerance without exceeding your actuator's limits.

---

## 3. How to Use the Tool

1.  **Generate Data:** Run the WPILib SysId logging routines on your robot to generate a `.wpilog` file. Run four tests: Quasistatic Forward, Quasistatic Reverse, Dynamic Forward, and Dynamic Reverse.
2.  **Load Data:** Open D-Bug SysId, drag and drop your `.wpilog` file into the UI, or click the "Open Log" button.
3.  **Analyze Feedforward:** Select the mechanism type (Elevator, Arm, Simple) and units. SysId will instantly graph the velocity and acceleration responses and calculate $K_s, K_g, K_v,$ and $K_a$.
4.  **Analyze Feedback:** Scroll down to the Optimal Control (LQR) section. Adjust your Max Position Error, Max Velocity Error, and Max Control Effort sliders until you find a balance that suits your mechanism. 
5.  **Export Code:** Select your target framework (e.g., CTRE Phoenix 6) and copy the generated Code Snippet. The snippet will include properly scaled gains ready to paste directly into your robot code.

---

## 4. How This Helps the Programmer

*   **Eliminates Guesswork:** No more spending hours manually tweaking $K_p$ and $K_d$ while the robot oscillates wildly.
*   **Prevents Hardware Damage:** By defining maximum control effort in LQR, the programmer ensures the PID controller won't command aggressive voltage spikes that could strip gears or burn out motors.
*   **Unlocks Advanced Control:** Exposing Transfer Functions and State-Space matrices allows programmers to design custom controllers (like Model Predictive Control or LQR) in MATLAB or Python before writing robot code.
*   **Seamless Integration:** The built-in Code Snippet generator formats the math perfectly for libraries like CTRE Motion Magic, scaling the loop periods automatically (e.g., $1000\text{Hz}$ loops).

---

## 5. Next Steps: Implementing the Mechanism

Once you have extracted the data and copied the Code Snippet, follow these steps to implement the control logic on your robot:

1.  **Apply the Configuration:** Paste the generated CTRE `TalonFXConfiguration` or WPILib `Feedforward` object into your subsystem's initialization method.
2.  **Set the Control Mode:** When commanding the motor to move, use a closed-loop control request. For CTRE, use `MotionMagicVoltage` or `PositionVoltage`. 
3.  **Provide the Setpoint:** Pass the desired target position to the control request. The motor controller will now use your calculated $K_s, K_v, K_a,$ and $K_g$ to plan a trajectory and your LQR-tuned $K_p$ and $K_d$ to correct any disturbances.
4.  **Validate on Hardware:** Command the mechanism to move between two setpoints. Use AdvantageScope or FRC Web Components to graph the *Target Position* vs. *Actual Position*. If the lines overlap perfectly, your system identification was a success! If there is a slight steady-state error, consider a minimal $K_i$ gain or verify the physical weight hasn't changed.
