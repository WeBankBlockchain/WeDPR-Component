# -*- coding: utf-8 -*-

import threading


class ThreadSafeList:
    def __init__(self):
        self.processing_list = []
        self.lock = threading.Lock()

    def append(self, object):
        try:
            self.lock.acquire()
            self.processing_list.append(object)
        except Exception as e:
            raise e
        finally:
            self.lock.release()

    def remove(self, object):
        try:
            self.lock.acquire()
            self.processing_list.remove(object)
        except Exception as e:
            raise e
        finally:
            self.lock.release()

    def contains(self, object):
        try:
            self.lock.acquire()
            return object in self.processing_list
        except Exception as e:
            raise e
        finally:
            self.lock.release()

    def get(self):
        try:
            copyed_list = []
            self.lock.acquire()
            copyed_list.append(self.processing_list)
            return copyed_list
        except Exception as e:
            raise e
        finally:
            self.lock.release()

    def get_element(self, i):
        try:
            self.lock.acquire()
            return self.processing_list[i]
        except Exception as e:
            raise e
        finally:
            self.lock.release()
