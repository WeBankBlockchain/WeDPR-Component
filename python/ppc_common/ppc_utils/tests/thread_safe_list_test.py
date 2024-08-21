# -*- coding: utf-8 -*-
import unittest
import threading
from ppc_common.ppc_utils.thread_safe_list import ThreadSafeList


if __name__ == '__main__':
    unittest.main()

thread_safe_list = ThreadSafeList()


def test(object, thread_name, ut_obj):
    try:
        print(f"### thread: {thread_name}")
        # append
        thread_safe_list.append(object)
        # contains
        ut_obj.assertTrue(thread_safe_list.contains(object))
        # remove
        thread_safe_list.remove(object)
        ut_obj.assertTrue(thread_safe_list.contains(object) is False)
        print(f"### result: {thread_safe_list.get()}")
        # get element
        copied_list = thread_safe_list.get()
        if len(copied_list) >= 1:
            thread_safe_list.get_element(len(copied_list) - 1)
        thread_safe_list.get_element(len(copied_list))
    except Exception as e:
        print(f"### Exception: {e}")


class TestThreadSafeList(unittest.TestCase):
    def test_multi_thread(self):
        loops = 5
        for j in range(loops):
            thread_list = []
            thread_num = 20
            for i in range(thread_num):
                thread_name = "t" + str(i) + "_" + str(j)
                object = "job_" + str(j) + "_" + str(i)
                t = threading.Thread(target=test, name=thread_name, args=(object,
                                                                          thread_name, self))
                thread_list.append(t)
            for t in thread_list:
                t.start()
            for t in thread_list:
                t.join()
