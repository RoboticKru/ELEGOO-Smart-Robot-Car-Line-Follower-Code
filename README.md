# ELEGOO-Smart-Robot-Car-Line-Follower-Code

 # **1. The Core Philosophy**

Most basic line followers use a simple "if/else" structure (e.g., "If I see white, turn left"). This fails spectacularly on complex tracks (gaps, sharp curves, variable lighting).

Our code uses a State Machine architecture paired with Dynamic Sensor Tuning and Inertial Measurement (IMU) to create a robot that "understands" its momentum and environment.

# **2. Phase 1: Startup & Auto-Calibration**

The robot does not use hardcoded light thresholds. Light changes based on room lighting and floor material.

The "Wiggle": When powered on, the robot waits 2 seconds (safety delay), then spins left and right.

The Math: During this spin, it records the absolute darkest (Max) and lightest (Min) values it sees. It then calculates the perfect threshold for that specific track: Threshold = (Max + Min) / 2.

The Gyro: Simultaneously, it takes 1,000 readings from the Gyroscope to find the "zero" offset, ensuring rotation tracking is perfectly accurate.

# **3. Phase 2: Active Tracking (The PID Loop)**

When on the line, the robot uses a Proportional-Derivative (PID) Controller.

Proportional (P): How far off the line are we? (Steer harder the further we drift).

Derivative (D): How fast are we drifting? (This "dampens" the steering to prevent the robot from violently whipping back and forth).

Aggressive Tuning: We tuned Kp (Proportional) very high so the robot "snaps" to sharp curves instantly.

# **4. The "Secret Sauce": Gyro Stability Lock**

This is the feature that prevents the robot from driving off the track on 90° or 180° turns.

The Problem: On a sharp turn, the front sensors cross the line, tell the wheels "We are centered, go straight!", but the body of the robot is still sideways. The robot shoots off the track.

The Solution: We read the Z-axis (Yaw) rotation speed from the MPU6050 Gyroscope. If the sensors say "Center" BUT the Gyro says "We are still spinning really fast," the robot applies the brakes for 50ms. It waits for the chassis to align with the line before accelerating.

# **5. Phase 3: The "Hunter-Seeker" Rescue Protocol**

If the robot completely loses the line (sensors see all white), it does not reverse. Reversing often causes the robot to back up perpendicularly into the line, tricking the sensors.

Instead, it enters a tiered rescue state:

Momentum Carry (0-150ms): If the line vanishes, it assumes it's a gap in the track. It drives blindly forward for a fraction of a second, letting momentum carry it over the gap.

The Sweep (The Hunter-Seeker): If the line doesn't return, it hits the brakes. It remembers the last known direction (e.g., "The line was on my left").

Gyro-Locked Search: It spins in place exactly 135 degrees towards the last known side. If it finds nothing, it sweeps back 270 degrees to check the other side. (We restricted it to 135° to prevent it from spinning a full 180° and driving backward down the track).

The Mini-Move: If the sweep fails, it drives forward blindly for 300ms to change its vantage point, then sweeps again. It tries this 3 times before giving up.

# **6. Phase 4: Lift Detection (Safety)**

We use the Z-axis Accelerometer to monitor gravity. If the AcZ value drops significantly, it means the robot has been picked up (or fell). It immediately kills power to the motors and waits until it detects a stable landing before resuming.
