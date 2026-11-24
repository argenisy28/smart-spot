# Tools

Development tools for Smart Spot, such as the device simulator.
Owners: Argenis, Zapata.


## Device Simulator

The Smart Spot Device Simulator is a Python-based development tool that simulates parking occupancy and device communication states without requiring physical hardware.

### Features

- Simulation of multiple parking spaces.
- Random ultrasonic sensor distance measurements.
- Parking occupancy detection.
- State-change-only reporting.
- Simulated device disconnection and recovery.
- Automatic occupancy reporting after recovery.
- Automated unit testing.

### Requirements

- Python 3 (tested with Python 3.13.3).
- No external Python dependencies.

### Running the Simulator

From the repository root, execute:

```powershell
python tools/device_simulator.py
```

Press Ctrl+C to stop the simulator.

### Simulator Configuration

| Parameter | Default Value |
|---|---|
| Number of parking spaces | 4 |
| Detection threshold | 20 cm |
| Update interval | 2 seconds |
| Failure probability per cycle | 5% |
| Recovery probability per cycle | 30% |

These values are provisional and subject to confirmation by the development team.

### Running Automated Tests

From the repository root, execute:

```powershell
python -m unittest discover -s tools/tests -v
```

### Testing Results

All eight automated unit tests passed successfully.

The tests verify occupancy detection, detection thresholds, sensor measurement ranges, device disconnection, recovery, and communication states.

### Current Limitations

- REST API communication is not implemented.
- MySQL database integration is not implemented.
- Physical hardware communication is not implemented.
- Device report formats are pending confirmation.
- Each parking space is associated with one simulated device.

### Future Integration

The simulator will be adapted to support the official Smart Spot API contract, device reports, device health monitoring, and backend integration testing once the communication requirements are confirmed.
