import time
from prefect import Task, Flow
from prefect.triggers import all_successful, any_failed
from prefect import Flow
from prefect.executors import LocalDaskExecutor

class SuccessTask(Task):
    def run(self):
        print(" ===>>>> Success Task Ran")

class FailTask(Task):
    def run(self):
        print(" ===>>> Fail Task Ran")

class JobTask(Task):
    def __init__(self, name):
        super().__init__(name=name)
        self.name = name
    
    def run(self, x, y):
        print(" ==> " + self.name + " is running, " + str(x) + ", " + str(y))
        return x + y

# with Flow("My Flow") as f:
#     t1 = a(1, 2) # t1 != a
#     t2 = a(5, 7) # t2 != a

with Flow("example_flow") as flow:
    
    task1 = JobTask("t1")
    task2 = JobTask("t2")
    task3 = JobTask("t3")
    task4 = JobTask("t4")
    
    # task1=t1(1,2)
    # task2=t2(3,4)
    # task3=t3(4,5)
    # task4=t4(6,7)
    task1.bind(x = 1, y = 2)
    task2.bind(x = 3, y = 4)
    task3.bind(x = 5, y = 6)
    task4.bind(x = 7, y = 8)

    task3.set_upstream(task1, flow=flow)
    task3.set_upstream(task2, flow=flow)
    task4.set_upstream(task3, flow=flow)
    
    # 定义成功任务，仅当所有上游任务成功时才运行
    success_task = SuccessTask(name="Success Task")
    success_task.trigger = all_successful
    
    # 定义失败任务，只要任何一个上游任务失败就运行
    fail_task = FailTask(name="Fail Task")
    fail_task.trigger = any_failed
    
    # 设置依赖关系
    task1.set_downstream(success_task, flow=flow)
    task2.set_downstream(success_task, flow=flow)
    task3.set_downstream(success_task, flow=flow)
    task4.set_downstream(success_task, flow=flow)
    
    task1.set_downstream(fail_task, flow=flow)
    task2.set_downstream(fail_task, flow=flow)
    task3.set_downstream(fail_task, flow=flow)
    task4.set_downstream(fail_task, flow=flow)


# flow.executor = LocalDaskExecutor()

start_time = time.time()
# 运行工作流
flow_state=flow.run()

end_time = time.time()
print(f" ## costs: {end_time - start_time}")

print(flow_state.result)

flow.visualize(flow_state, "./my_flow", 'svg')