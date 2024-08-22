import os
import threading

from ppc_model.common.initializer import Initializer

dirName, _ = os.path.split(os.path.abspath(__file__))
# config_path = '{}/../application.yml'.format(dirName)
config_path = "application.yml"

components = Initializer(
    log_config_path='logging.conf', config_path=config_path)

# matplotlib 线程不安全，并行任务绘图增加全局锁
plot_lock = threading.Lock()
