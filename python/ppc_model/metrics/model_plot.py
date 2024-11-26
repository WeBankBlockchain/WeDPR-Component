import gc
import time
import random
import traceback
import matplotlib.pyplot as plt
import networkx as nx
from networkx.drawing.nx_pydot import graphviz_layout

from ppc_model.common.model_result import ResultFileHandling
from ppc_model.secure_lgbm.vertical.booster import VerticalBooster


class ModelPlot:

    def __init__(self, model: VerticalBooster) -> None:

        self.ctx = model.ctx
        self.model = model
        self._tree_id = 0
        self._leaf_id = None
        self._G = None
        self._split = None
        self.storage_client = self.ctx.components.storage_client

        if model._trees is not None and \
                self.ctx.components.config_data['AGENCY_ID'] in self.ctx.result_receiver_id_list:
            self.plot_tree()

    def plot_tree(self):
        trees = self.model._trees
        self._split = self.model._X_split

        for i, tree in enumerate(trees):
            if i < 6:
                tree_file_path = self.ctx.model_tree_prefix + \
                    '_' + str(self._tree_id)+'.svg'
                remote_tree_file_path = self.ctx.remote_model_tree_prefix + \
                    '_' + str(self._tree_id)+'.svg'
                self._tree_id += 1
                self._leaf_id = 0
                self._G = DiGraphTree()
                if not isinstance(tree, list) or tree == 0:
                    continue
                else:
                    self._graph_gtree(tree)

                max_retry = 3
                retry_num = 0
                while retry_num < max_retry:
                    retry_num += 1
                    try:
                        with self.ctx.components.plot_lock:
                            self._G.tree_plot(
                                figsize=(10, 5), save_filename=tree_file_path)
                    except:
                        self.ctx.components.logger().info(
                            f'tree_id = {i}, tree = {tree}')
                        self.ctx.components.logger().info(f'G = {self._G}')
                        err = traceback.format_exc().replace('\n', ' ; error: ')
                        # self.ctx.components.logger().exception(err)
                        self.ctx.components.logger().info(
                            f'plot tree-{i} in times-{retry_num} failed, traceback: {err}.')
                        time.sleep(random.uniform(0.1, 3))

                ResultFileHandling._upload_file(
                    self.storage_client, tree_file_path,
                    remote_tree_file_path, self.ctx.user)

    def _graph_gtree(self, tree, leaf_id=0, depth=0, orient=None, split_info=None):
        self._leaf_id += 1
        self._G.add_node(self._leaf_id)
        if split_info is not None:
            if self.ctx.participant_id_list[split_info.agency_idx] == self.ctx.components.config_data['AGENCY_ID']:
                feature = str(
                    self.model.dataset.feature_name[split_info.agency_feature])
                value = str(
                    round(float(self._split[split_info.agency_feature][split_info.value]), 4))
            else:
                feature = str(split_info.feature)
                value = str(split_info.value)
        else:
            feature = value = ''

        if isinstance(tree, list):
            best_split_info, left_tree, right_tree = tree[0]
            if leaf_id != 0:
                if orient == 'left':
                    self._G.add_weighted_edges_from(
                        [(leaf_id, self._leaf_id, orient+'_'+feature+'_'+value+'_'+str(split_info.w_left))])
                elif orient == 'right':
                    self._G.add_weighted_edges_from(
                        [(leaf_id, self._leaf_id, orient+'_'+feature+'_'+value+'_'+str(split_info.w_right))])
            my_leaf_id = self._leaf_id
            self._graph_gtree(left_tree, my_leaf_id, depth +
                              1, 'left', best_split_info)
            self._graph_gtree(right_tree, my_leaf_id, depth +
                              1, 'right', best_split_info)
        else:
            if leaf_id != 0:
                self._G.add_weighted_edges_from(
                    [(leaf_id, self._leaf_id, orient+'_'+feature+'_'+value+'_'+str(tree))])


class DiGraphTree(nx.DiGraph):

    def __init__(self):

        super().__init__()

    def tree_leaves(self):
        leaves_list = [x for x in self.nodes() if self.out_degree(
            x) == 0 and self.in_degree(x) <= 1]
        return leaves_list

    def tree_dfs_nodes(self):
        nodes_list = list(nx.dfs_preorder_nodes(self))
        return nodes_list

    def tree_dfs_leaves(self):
        dfs_leaves = [x for x in self.tree_dfs_nodes()
                      if x in self.tree_leaves()]
        return dfs_leaves

    def tree_depth(self):
        max_depth = max(nx.shortest_path_length(self, 0).values())
        return max_depth

    def tree_shortest_path(self, node0, node1):
        path_length = nx.shortest_path_length(self, node0, node1)
        return path_length

    def tree_plot(self, split=True, figsize=(20, 10), dpi=300, save_filename=None):
        # plt.cla()
        pos = graphviz_layout(self, prog='dot')
        # pos = nx.nx_agraph.graphviz_layout(self, prog='dot')
        edge_labels = nx.get_edge_attributes(self, 'weight')

        if split:
            labels = {}
            # leaves = self.tree_leaves()
            leaves = [x for x in self.nodes() if self.out_degree(x) ==
                      0 and self.in_degree(x) <= 1]

            if leaves == [0]:
                leaves = []
                self.remove_node(0)

            for n in self.nodes():

                if n in leaves:
                    # in_node = list(nx.all_neighbors(self, n))[0]
                    in_node = list(self.predecessors(n))[0]
                    weight = edge_labels[(in_node, n)]
                    try:
                        labels[n] = round(float(str(weight).split('_')[3]), 4)
                    except:
                        labels[n] = str(weight).split('_')[3]
                else:
                    in_node = list(nx.neighbors(self, n))[0]
                    weight = edge_labels[(n, in_node)]
                    labels[n] = weight.split(
                        '_')[1] + ':' + weight.split('_')[2]

            # for key, value in edge_labels.items():
            #     edge_labels[key] = round(float(value.split('_')[3]), 4)

            plt.figure(figsize=figsize, dpi=dpi)
            nx.draw(self, pos,
                    node_size=1000, node_color='#72BFC5', node_shape='o', alpha=None,
                    with_labels=True, labels=labels, font_weight='normal', font_color='black')
            # nx.draw_networkx_edge_labels(self, pos, edge_labels=edge_labels)
            # plt.show()
            if save_filename is not None:
                plt.savefig(save_filename)
            else:
                plt.show()

        else:
            labels = {n: n for n in self.nodes()}
            for key, value in edge_labels.items():
                edge_labels[key] = value.split('_')[1] + '-' + value.split('_')[2] + \
                    '-' + str(round(float(value.split('_')[3]), 4))

            plt.figure(figsize=figsize, dpi=dpi)
            nx.draw(self, pos, with_labels=True,
                    labels=labels, font_weight='bold')
            nx.draw_networkx_edge_labels(self, pos, edge_labels=edge_labels)
            # plt.show()
            if save_filename is not None:
                plt.savefig(save_filename)
            else:
                plt.show()

        plt.close('all')
        gc.collect()
