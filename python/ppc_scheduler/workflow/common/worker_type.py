class WorkerType:
    # generic job worker
    T_API = 'API'
    T_PYTHON = 'PYTHON'
    T_SHELL = 'SHELL'

    # specific job worker
    T_PSI = 'PSI'
    T_MPC = 'MPC'
    T_MODEL = "MODEL"

    # finish job
    T_ON_SUCCESS = 'T_ON_SUCCESS'
    T_ON_FAILURE = 'T_ON_FAILURE'
