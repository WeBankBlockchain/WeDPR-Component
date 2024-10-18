# -*- coding: utf-8 -*-

class Properties:
    def __init__(self, file_path):
        self.file_path = file_path

    def getProperties(self):
        try:
            pro_file = open(self.file_path, 'r', encoding='utf-8')
            properties = {}
            for line in pro_file:
                if line.find('=') > 0:
                    strs = line.strip("\"").replace('\n', '').split('=')
                    properties[strs[0].strip()] = strs[1].strip()
        except Exception as e:
            raise e
        else:
            pro_file.close()
        return properties
