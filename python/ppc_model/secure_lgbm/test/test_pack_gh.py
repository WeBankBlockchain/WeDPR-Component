import unittest
import numpy as np

from ppc_model.secure_lgbm.vertical.booster import VerticalBooster


class TestPackGH(unittest.TestCase):

    def test_pack_gh(self):

        g_list = np.array([-4, 2, -1.3, 0, 0, -15.3544564544])
        h_list = np.array([2, 1.5, -1.4, 0, -1.68, 1.2356564564])

        gh_list = VerticalBooster.packing_gh(g_list, h_list)

        result_array = np.array(
            [429496329600000000000000002000, 200000000000000000001500,
             429496599600000000004294965896, 0,
             4294965616, 429495194200000000000000001235], dtype=object)

        assert np.array_equal(gh_list, result_array)

    def test_unpack_gh(self):

        gh_list = np.array(
            [429496329600000000000000002000, 200000000000000000001500,
             429496599600000000004294965896, 0,
             4294965616, 429495194200000000000000001235], dtype=object)

        gh_sum_list = np.array([sum(gh_list), sum(gh_list)*2])
        g_hist, h_hist = VerticalBooster.unpacking_gh(gh_sum_list)

        g_list = np.array([-4, 2, -1.3, 0, 0, -15.3544564544])
        h_list = np.array([2, 1.5, -1.4, 0, -1.68, 1.2356564564])
        result_g_hist = np.array([-18.654, -37.308])
        result_h_hist = np.array([1.655, 3.31])

        assert np.array_equal(g_hist, result_g_hist)
        assert np.array_equal(h_hist, result_h_hist)


if __name__ == '__main__':
    unittest.main()
