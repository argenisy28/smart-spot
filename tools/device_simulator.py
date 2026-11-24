
"""
Smart Spot - Device Simulator

Simulates parking occupancy, ultrasonic sensor
measurements, device disconnections and recovery.
"""

import random
import time


# Simulator configuration
NUMBER_OF_SPOTS = 4
DETECTION_THRESHOLD_CM = 20.0
UPDATE_INTERVAL = 2

FAILURE_PROBABILITY = 0.05
RECOVERY_PROBABILITY = 0.30


# Parking occupancy states
parking_states = {
    spot_id: None
    for spot_id in range(1, NUMBER_OF_SPOTS + 1)
}


# Device communication states
device_health = {
    spot_id: True
    for spot_id in range(1, NUMBER_OF_SPOTS + 1)
}


def simulate_distance():
    """Generate a simulated ultrasonic sensor reading."""
    return round(random.uniform(5.0, 100.0), 1)


def detect_occupancy(distance):
    """Determine whether a parking spot is occupied."""
    return distance <= DETECTION_THRESHOLD_CM


def simulate_device_health(spot_id):
    """Simulate device disconnection and recovery."""

    if device_health[spot_id]:

        if random.random() < FAILURE_PROBABILITY:
            device_health[spot_id] = False

            print(f"Device {spot_id}: OFFLINE")

    else:

        if random.random() < RECOVERY_PROBABILITY:
            device_health[spot_id] = True

            # Force a new occupancy report after recovery
            parking_states[spot_id] = None

            print(f"Device {spot_id}: RECOVERED")

    return device_health[spot_id]


def simulate_parking():
    """Continuously simulate parking and device activity."""

    print("=" * 45)
    print("Smart Spot - Device Simulator")
    print("System Started")
    print("=" * 45)

    try:
        while True:

            for spot_id in parking_states:

                # Check device communication status
                if not simulate_device_health(spot_id):
                    continue

                # Generate sensor measurement
                distance = simulate_distance()
                occupied = detect_occupancy(distance)

                # Report only when occupancy changes
                if parking_states[spot_id] != occupied:

                    parking_states[spot_id] = occupied

                    if occupied:
                        print(
                            f"Parking Spot {spot_id}: "
                            f"OCCUPIED - Distance: {distance} cm"
                        )
                    else:
                        print(f"Parking Spot {spot_id}: FREE")

            time.sleep(UPDATE_INTERVAL)

    except KeyboardInterrupt:
        print("\nDevice simulator stopped.")


if __name__ == "__main__":
    simulate_parking()
