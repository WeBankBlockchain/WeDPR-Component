import numpy as np


class Sampling:

    @staticmethod
    def sample_selecting(
        g_list: np.ndarray,
        h_list: np.ndarray,
        subsample: float = 0,
        use_goss: bool = False,
        top_rate: float = 0.2,
        other_rate: float = 0.1
    ):
        if use_goss:
            instance, used_glist, used_hlist = Sampling._get_goss_sampling(
                g_list, h_list, top_rate, other_rate)
        elif subsample > 0 and subsample < 1:
            instance, used_glist, used_hlist = Sampling._get_subsample_sampling(
                g_list, h_list, subsample)
        else:
            instance, used_glist, used_hlist = Sampling._get_sampling(
                g_list, h_list)

        return instance, used_glist, used_hlist

    def _get_goss_sampling(g_list, h_list, top_rate, other_rate):

        n = len(g_list)
        instance, used_glist, used_hlist = Sampling._goss_sampleing(
            n, top_rate, other_rate, g_list, h_list)

        return instance, used_glist, used_hlist

    def _get_subsample_sampling(g_list, h_list, subsample):

        rand_size = int(len(g_list) * subsample)
        rand_idx = np.array(sorted(
            np.random.choice(list(range(len(g_list))), size=rand_size, replace=False)))
        used_glist = np.array(g_list)[(rand_idx)]
        used_hlist = np.array(h_list)[(rand_idx)]

        # used_idx = {}
        # for i in range(rand_size):
        #     used_idx[rand_idx[i]] = i
        # curr_instance = np.array(list(used_idx.keys()))

        return rand_idx, used_glist, used_hlist

    def _get_sampling(g_list, h_list):

        used_glist, used_hlist = g_list, h_list
        instance = np.array(list(range(len(g_list))))

        return instance, used_glist, used_hlist

    @staticmethod
    def _goss_sampleing(n, a, b, g_list, h_list):
        top_size = int(n * a)
        rand_size = int(n * b)
        abs_g = np.abs(g_list)

        top_idx = np.argsort(abs_g)[-top_size:]
        rand_idx = np.random.choice(np.argsort(
            abs_g)[:-top_size], size=rand_size, replace=False)
        used_idx = np.append(top_idx, rand_idx)

        fact = (1 - a) / b
        rand_glist = np.array(g_list)[(rand_idx)] * fact
        used_glist = np.append(np.array(g_list)[(top_idx)], rand_glist)
        rand_hlist = np.array(h_list)[(rand_idx)] * fact
        used_hlist = np.append(np.array(h_list)[(top_idx)], rand_hlist)

        return Sampling._sort_instance(used_idx, used_glist, used_hlist)

    @staticmethod
    def _sort_instance(instance, g_list, h_list):
        # 获取排序索引
        sorted_indices = np.argsort(instance)

        # 对所有数组进行排序
        sorted_idx = instance[sorted_indices]
        sorted_glist = g_list[sorted_indices]
        sorted_hlist = h_list[sorted_indices]

        return sorted_idx, sorted_glist, sorted_hlist
