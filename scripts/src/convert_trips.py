from datetime import datetime, date, time
from collections import defaultdict
import os

import simdjson as json
import pandas as pd

from src.ts_types import Station, StopTime, Stop, Trip, Route
from src.utils import mkpath, dump_json


# TRIPS_COLUMNS = {
#     'route_id': str,
#     'trip_id': int
# }
STOP_TIMES_COLUMNS = {
    'trip_id': str,
    'stop_id': str,
    'arrival_time': str,
    'departure_time': str,
    'stop_sequence': 'uint8'
}


def convert_trips(raw_data_path: str, data_path: str, data_date: date):
    data_timestamp = int(datetime.combine(data_date, time(0, 0)).timestamp())
    print('Converting trips...')

    assert os.path.isfile(mkpath(data_path, 'stations.json')), 'Run convert_stations.py first'
    assert os.path.isfile(mkpath(data_path, 'stations_map.json')), 'Run convert_stations.py first'

    with open(mkpath(data_path, 'stations.json'), 'r', encoding='utf-8') as file:
        stations_by_id = {station.id: station for station in map(Station._make, json.load(file))}

    with open(mkpath(data_path, 'stations_map.json'), 'r', encoding='utf-8') as file:
        stations_by_id.update(
            (map_from, stations_by_id[map_to])
            for map_from, map_to in json.load(file).items()
        )

    print('Reading stop_times.csv...')
    stop_times_csv = pd.read_csv(
        mkpath(raw_data_path, 'stop_times.csv'),
        usecols=tuple(STOP_TIMES_COLUMNS.keys()),
        dtype=STOP_TIMES_COLUMNS,
        na_filter=False
    )

    stops_for_trip: dict[str, list[Stop]] = defaultdict(list)

    print(f'Parsing stop times ({len(stop_times_csv.index)} rows)...')

    stop_times_csv.sort_values('stop_sequence', inplace=True)
    stop_times_csv.drop(columns='stop_sequence', inplace=True)

    for row in stop_times_csv.itertuples(index=False):
        if row.stop_id not in stations_by_id:
            print(f'Stop {row.stop_id} not found')
            continue  # TODO

        arrival_hours, arrival_minutes, arrival_seconds = map(int, row.arrival_time.split(':'))
        departure_hours, departure_minutes, departure_seconds = map(int, row.departure_time.split(':'))

        arrival_days, arrival_hours = divmod(arrival_hours, 24)
        departure_days, departure_hours = divmod(departure_hours, 24)

        arrival = arrival_days * 86400 + arrival_hours * 3600 + arrival_minutes * 60 + arrival_seconds
        departure = departure_days * 86400 + departure_hours * 3600 + departure_minutes * 60 + departure_seconds

        stops_for_trip[row.trip_id].append(
            Stop(
                station=stations_by_id[row.stop_id],
                time=StopTime(data_timestamp + arrival, data_timestamp + departure)
            )
        )

    del stations_by_id
    del stop_times_csv

    print('Checking stop times...')
    for stops in stops_for_trip.values():  # Checking departure and arrival time consistency
        for i in range(1, len(stops)):
            assert stops[i - 1].time.departure_time <= stops[i].time.arrival_time, (
                f'Stop {stops[i - 1].station.id}/{stops[i - 1].station.name} ({stops[i - 1].time.departure_time}) '
                f'and stop {stops[i].station.id}/{stops[i].station.name} ({stops[i].time.arrival_time}) '
                f'are not in the correct order in trip {row.trip_id}'
            )

    print(f'Arranging trips by route...')

    similar_trips_dict: dict[tuple[Station], list[str]] = defaultdict(list)  # {stations: [trip_ids]}

    for trip_id, stops in stops_for_trip.items():
        similar_trips_dict[tuple(stop.station for stop in stops)].append(trip_id)

    similar_trips: Route = []

    for stations, trip_ids in similar_trips_dict.items():
        similar_trips.append(
            Route(
                station_ids=[station.id for station in stations],
                trips=[
                    Trip([
                        stop.time
                        for stop in stops_for_trip[trip_id]
                    ]) for trip_id in trip_ids
                ]
            )
        )

    del similar_trips_dict
    del stops_for_trip

    print('Total amount of routes:', len(similar_trips))
    print('Average amount of trips per route:', round(
        sum(len(route.trips) for route in similar_trips) / len(similar_trips)
    ))

    print('Writing result...')
    dump_json(similar_trips, mkpath(data_path, 'trips.json'), indent_depth=4)


if __name__ == '__main__':
    convert_trips()
