# ParkingLotManagement

## Abstract

## LotMonitor

LotMonitor monitors one specific parking place.

Functions:

- signal if the place is reserved or free to claim
- signal distance to sensor (optional)
- report status on request to the LotManager

## LotManager

LotManager controlles a number of LotMonitors.

Function:

- distribute reservations to LotMonitors
- query status from LotMonitors
- report status on change to the LotManagement

## LotManagement

Human friendly WebService for managing purposes.

Function:

- distribute reservations to LotManager(s)
- process status changes from LotManager(s)
- report status changes to communication channel(s)
