
"""
Smart Spot - Device Simulator Unit Tests
"""

import unittest
from unittest.mock import patch

from tools import device_simulator as simulator


class TestDeviceSimulator(unittest.TestCase):

    def setUp(self):
        """Reset device states before each test."""
        for spot_id in simulator.parking_states:
            simulator.parking_states[spot_id] = None
            simulator.device_health[spot_id] = True

    def test_occupied_parking_spot(self):
        self.assertTrue(
            simulator.detect_occupancy(10.0)
        )

    def test_free_parking_spot(self):
        self.assertFalse(
            simulator.detect_occupancy(50.0)
        )

    def test_detection_threshold(self):
        self.assertTrue(
            simulator.detect_occupancy(
                simulator.DETECTION_THRESHOLD_CM
            )
        )

    def test_distance_range(self):
        for _ in range(100):
            distance = simulator.simulate_distance()

            self.assertGreaterEqual(distance, 5.0)
            self.assertLessEqual(distance, 100.0)

    @patch("tools.device_simulator.random.random",
           return_value=0.0)
    def test_device_disconnection(self, mock_random):
        self.assertFalse(
            simulator.simulate_device_health(1)
        )

        self.assertFalse(simulator.device_health[1])

    @patch("tools.device_simulator.random.random",
           return_value=0.0)
    def test_device_recovery(self, mock_random):
        simulator.device_health[1] = False
        simulator.parking_states[1] = True

        self.assertTrue(
            simulator.simulate_device_health(1)
        )

        self.assertTrue(simulator.device_health[1])
        self.assertIsNone(simulator.parking_states[1])

    @patch("tools.device_simulator.random.random",
           return_value=0.99)
    def test_device_remains_offline(self, mock_random):
        simulator.device_health[1] = False

        self.assertFalse(
            simulator.simulate_device_health(1)
        )

    @patch("tools.device_simulator.random.random",
           return_value=0.99)
    def test_device_remains_online(self, mock_random):
        self.assertTrue(
            simulator.simulate_device_health(1)
        )


if __name__ == "__main__":
    unittest.main()
