# -*- coding: utf-8 -*-
import argparse
from enum import Enum
import datetime
import random
import string
import sys


class IDType(Enum):
    CreditID = "credit_id",

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_


class WithTimeType(Enum):
    Empty = "none",
    Random = "random",
    ALL = "all",

    @classmethod
    def has_value(cls, value):
        return value in cls._value2member_map_

    @classmethod
    def value_of(cls, label):
        if label in cls.Empty.value:
            return cls.Empty
        elif label in cls.Random.value:
            return cls.Random
        elif label in cls.ALL.value:
            return cls.ALL
        else:
            raise NotImplementedError


def parse_args():
    parser = argparse.ArgumentParser(prog=sys.argv[0])
    parser.add_argument("-r", '--dataset_file',
                        help='the file to store the faked data', required=True)
    parser.add_argument("-p", '--peer_dataset_size',
                        help='the peer dataset size', default=0, required=False)
    parser.add_argument("-j", '--join_id_count',
                        help='the joined id count', default=0, required=False)
    parser.add_argument("-c", '--id_count',
                        help='the id count', required=False)
    parser.add_argument("-f", '--id_file',
                        help='the id file', required=False)

    parser.add_argument("-t", '--with_time',
                        help=f'generate id information with time({WithTimeType.Empty.value} means not with time, random means only generate a random time for one id, all means generate (end_date-start_date) times for one id)', default=WithTimeType.Empty.value[0], required=False)

    parser.add_argument("-s", '--start_date',
                        help='the start time(only useful when --with_time is setted)', required=False)
    parser.add_argument("-e", '--end_date',
                        help='the end time(only useful when --with_time is setted)', required=False)
    parser.add_argument("-I", '--id_type',
                        help=f'the id type, currently only support {IDType.CreditID.value}',
                        default=IDType.CreditID.value, required=False)
    args = parser.parse_args()
    return args


def generate_credit_id():
    code = random.choice(string.digits + string.ascii_uppercase)
    code += random.choice(string.digits + string.ascii_uppercase)
    code += ''.join(random.choices(string.digits, k=6))
    code += ''.join(random.choices(string.digits, k=9))
    code += random.choice(string.digits + string.ascii_uppercase)
    return code


def generate_id(id_type):
    if IDType.has_value(id_type) is False:
        error_msg = f"Unsupported id type: {id_type}"
        raise Exception(error_msg)
    id_type = IDType(id_type)
    if id_type == IDType.CreditID:
        return generate_credit_id()


def generate_random_time(start_date, end_date):
    start_time = datetime.time(8, 0, 0)
    random_date = start_date + \
        (end_date - start_date) * random.random()
    random_time = datetime.datetime.combine(
        random_date, start_time) + datetime.timedelta(seconds=random.randint(0, 86399))
    return random_time.strftime("%Y-%m-%d")


def write_line_data(fp, with_time, id_data, start_date, end_date):
    if with_time is WithTimeType.Random:
        line_data = id_data + "," + \
            generate_random_time(start_date, end_date) + "\n"
        fp.write(line_data)
    elif with_time is WithTimeType.ALL:
        for i in range((end_date - start_date).days + 1):
            day = start_date + datetime.timedelta(days=i)
            line_data = id_data + "," + day.strftime("%Y-%m-%d") + "\n"
            fp.write(line_data)
    else:
        fp.write(id_data + "\n")


def generate_header(dataset_file, with_time):
    with open(dataset_file, "a+") as fp:
        # with the header
        fp.write("id")
        if with_time is not WithTimeType.Empty:
            fp.write(",time")
        fp.write("\n")


def generate_dataset(id_count, id_type, dataset_file, with_time, start_date, end_date, joined_fp, joined_count, id_file):
    with open(dataset_file, "a+") as f:
        # generate with id_count
        if id_file is None:
            epoch = int(id_count) * 0.1
            for i in range(int(id_count)):
                if i % epoch == 0:
                    print(
                        f"#### generate {epoch}/{id_count} records for: {dataset_file}")
                id_data = generate_id(id_type)
                write_line_data(f, with_time, id_data, start_date, end_date)
                if joined_fp is not None and joined_count > 0:
                    joined_count -= 1
                    write_line_data(joined_fp, with_time,
                                    id_data, start_date, end_date)
        # generate with id_file
        else:
            with open(id_file, "r") as id_fp:
                # skip the header
                id_data = id_fp.readline().strip()
                if id_data is not None:
                    id_data = id_fp.readline().strip()
                while id_data is not None and id_data != '':
                    write_line_data(f, with_time, id_data,
                                    start_date, end_date)
                    if joined_fp is not None and joined_count > 0:
                        joined_count -= 1
                        write_line_data(joined_fp, with_time,
                                        id_data, start_date, end_date)
                    id_data = id_fp.readline().strip()

    print(f"##### fake_data for {dataset_file} success")


def check_args(args):
    if args.id_count is None and args.id_file is None:
        raise Exception("Must set id_count or id_file")


def fake_data(args):
    check_args(args)
    end_date = datetime.datetime.now()
    if args.end_date is not None:
        end_date = datetime.datetime.strptime(args.end_date, "%Y-%m-%d")
    start_date = datetime.datetime.now() + datetime.timedelta(days=-2 * 365)
    if args.start_date is not None:
        start_date = datetime.datetime.strptime(args.start_date, "%Y-%m-%d")
    with_time = WithTimeType.Empty
    if args.with_time is not None:
        with_time = WithTimeType.value_of(args.with_time)

    joined_dataset_path = args.dataset_file + ".peer"
    joined_dataset_fp = None
    if int(args.join_id_count) > 0:
        generate_header(joined_dataset_path, with_time)
        if int(args.peer_dataset_size) > int(args.join_id_count):
            expected_peer_data_size = int(
                args.peer_dataset_size) - int(args.join_id_count)
            generate_dataset(expected_peer_data_size, args.id_type,
                             joined_dataset_path, with_time, start_date, end_date, None, 0, None)
        joined_dataset_fp = open(joined_dataset_path, "a+")

    generate_header(args.dataset_file, with_time)
    generate_dataset(args.id_count, args.id_type,
                     args.dataset_file, with_time, start_date, end_date, joined_dataset_fp, int(args.join_id_count), args.id_file)
    if joined_dataset_fp is not None:
        joined_dataset_fp.close()


if __name__ == "__main__":
    args = parse_args()
    fake_data(args)
