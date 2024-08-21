import json
import unittest

LEGAL_OPERATOR_LIST = ['>', '<', '>=', '<=', '==', '!=']
LEGAL_QUOTA_LIST = ['and', 'or']


def get_rule_detail(match_module_dict):
    ruleset = match_module_dict['ruleset']
    operator_set = set()
    count_set = set()
    quota_set = set()
    for ruleset_item in ruleset:
        sub_rule_set = ruleset_item['set']
        for sub_rule_item in sub_rule_set:
            rule = sub_rule_item['rule']
            operator_set.add(rule['operator'])
            count_set.add(rule['count'])
            quota_set.add(rule['quota'])
    return operator_set, count_set, quota_set


def check_dataset_id_has_duplicated(match_module_dict):
    ruleset = match_module_dict['ruleset']
    for ruleset_item in ruleset:
        sub_rule_set = ruleset_item['set']
        for sub_rule_item in sub_rule_set:
            dataset_id_set = set()
            dataset_id_list = sub_rule_item['dataset']
            for dataset_id in dataset_id_list:
                dataset_id_set.add(dataset_id)
            if len(dataset_id_set) != len(dataset_id_list):
                return True
    return False


def get_dataset_id_set(match_module_dict):
    dataset_id_set = set()
    ruleset = match_module_dict['ruleset']
    for ruleset_item in ruleset:
        sub_rule_set = ruleset_item['set']
        for sub_rule_item in sub_rule_set:
            dataset_id_list = sub_rule_item['dataset']
            for dataset_id in dataset_id_list:
                dataset_id_set.add(dataset_id)
    return dataset_id_set


# get field_dataset_map: {'x1':['d1', 'd2', 'd3'], 'x2':['d1', 'd2', 'd3']}
def parse_field_dataset_map(match_module_dict):
    field_dataset_map = {}
    ruleset = match_module_dict['ruleset']
    for ruleset_item in ruleset:
        field = ruleset_item['field']
        if field in field_dataset_map:
            field_dataset_id_set = field_dataset_map[field]
        else:
            field_dataset_id_set = set()
            field_dataset_map[field] = field_dataset_id_set
        sub_rule_set = ruleset_item['set']
        for sub_rule_item in sub_rule_set:
            dataset_id_list = sub_rule_item['dataset']
            for dataset_id in dataset_id_list:
                field_dataset_id_set.add(dataset_id)
    return field_dataset_map


# get dataset_field_map: {'d1':['x1', 'x2'], 'd2':['x1', 'x2'], 'd3':['x1', 'x2']}
def parse_dataset_field_map(dataset_id_set, field_dataset_map):
    dataset_field_map = {}
    for dataset_id in dataset_id_set:
        for field, field_dataset_id_set in field_dataset_map.items():
            if dataset_id in field_dataset_id_set:
                if dataset_id in dataset_field_map:
                    dataset_field_set = dataset_field_map[dataset_id]
                else:
                    dataset_field_set = set()
                    dataset_field_map[dataset_id] = dataset_field_set
                dataset_field_set.add(field)
    return dataset_field_map


def parse_match_param(match_fields, match_module_dict):
    # step1 get field_dataset_map: {'x1':['d1', 'd2', 'd3'], 'x2':['d1', 'd2', 'd3']}
    field_dataset_map = parse_field_dataset_map(match_module_dict)
    # step2 get all dataset_id {'d1', 'd2', 'd3'}
    dataset_id_set = set()
    for field, field_dataset_id_set in field_dataset_map.items():
        dataset_id_set.update(field_dataset_id_set)
    # step3 get dataset_field_map: {'d1':['x1', 'x2'], 'd2':['x1', 'x2'], 'd3':['x1', 'x2']}
    dataset_field_map = parse_dataset_field_map(
        dataset_id_set, field_dataset_map)
    # step4 get match_param_list from dataset_field_map and match_field:
    # [
    # {'dataset_id':'d1', 'match_field':{'x1':'xxx, 'x2':'xxx},
    # {'dataset_id':'d2', 'match_field':{'x1':'xxx, 'x2':'xxx},
    # {'dataset_id':'d3', 'match_field':{'x1':'xxx, 'x2':'xxx},
    # ]
    match_param_list = parse_match_param_list(dataset_field_map, match_fields)
    return dataset_id_set, match_param_list


# get match_param_list from dataset_field_map and match_field:
# [
# {'dataset_id':'d1', 'match_field':{'x1':'xxx, 'x2':'xxx},
# {'dataset_id':'d2', 'match_field':{'x1':'xxx, 'x2':'xxx},
# {'dataset_id':'d3', 'match_field':{'x1':'xxx, 'x2':'xxx},
# ]
def parse_match_param_list(dataset_field_map, match_fields):
    match_param_list = []
    match_fields = match_fields.replace("'", '"')
    match_fields_object = json.loads(match_fields)
    for dataset_id, field_set in dataset_field_map.items():
        match_param = {'dataset_id': dataset_id}
        field_value_map = {}
        for field in field_set:
            # allow some part field match
            if field in match_fields_object.keys():
                field_value_map[field] = match_fields_object[field]
        match_param['match_field'] = field_value_map
        match_param_list.append(match_param)
    return match_param_list


class TestCemUtils(unittest.TestCase):
    def test_cem_match_algorithm_load(self):
        match_module = '{"ruleset":[' \
                       '{"field":"x1",' \
                       '"set":[' \
                       '{"rule":{"operator":"<","count":50,"quota":"and"},"dataset":["d1-encrypted","d2-encrypted"]},' \
                       '{"rule":{"operator":">","count":3,"quota":"or"},"dataset":["d3-encrypted"]}]},' \
                       '{"field":"x2","set":[' \
                       '{"rule":{"operator":"<","count":2,"quota":"or"},"dataset":["d1-encrypted","d2-encrypted","d3-encrypted"]}]}]}'
        match_module_dict = json.loads(match_module)
        # match_module_dict = utils.json_loads(match_module)
        print(match_module_dict)

    def test_check_dataset_id_has_duplicated(self):
        match_module = '{"ruleset":[' \
                       '{"field":"x1",' \
                       '"set":[' \
                       '{"rule":{"operator":"<","count":50,"quota":"and"},"dataset":["d1-encrypted","d2-encrypted"]},' \
                       '{"rule":{"operator":">","count":3,"quota":"or"},"dataset":["d3-encrypted"]}]},' \
                       '{"field":"x2","set":[' \
                       '{"rule":{"operator":"<","count":2,"quota":"or"},"dataset":["d1-encrypted","d2-encrypted","d3-encrypted"]}]}]}'
        match_module_dict = json.loads(match_module)
        has_duplicated = check_dataset_id_has_duplicated(match_module_dict)
        assert has_duplicated == False

    def test_check_dataset_id_has_duplicated(self):
        match_module = '{"ruleset":[' \
                       '{"field":"x1",' \
                       '"set":[' \
                       '{"rule":{"operator":"<","count":50,"quota":"and"},"dataset":["d1-encrypted","d2-encrypted"]},' \
                       '{"rule":{"operator":">","count":3,"quota":"or"},"dataset":["d3-encrypted"]}]},' \
                       '{"field":"x2","set":[' \
                       '{"rule":{"operator":"<","count":2,"quota":"or"},"dataset":["d1-encrypted","d1-encrypted","d3-encrypted"]}]}]}'
        match_module_dict = json.loads(match_module)
        has_duplicated = check_dataset_id_has_duplicated(match_module_dict)
        assert has_duplicated == True
