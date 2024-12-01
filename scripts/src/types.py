from typing import NamedTuple
from typing import NewType


class Station(NamedTuple):
    name: str
    id: str
    lat: float
    lon: float


class StopTime(NamedTuple):
    arrival_time: tuple[int, int, int]
    departure_time: tuple[int, int, int]


class Stop(NamedTuple):
    station: Station
    time: StopTime


Trip = NewType('Trip', list[StopTime])


class Route(NamedTuple):
    stations: list[Station]
    trips: list[Trip]
