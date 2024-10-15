class WorkerType:
    # generic job worker
    T_API = 'API'
    T_PYTHON = 'PYTHON'
    T_SHELL = 'SHELL'

    # specific job worker
    T_PSI = 'PSI'
    T_ML_PSI = 'ML_PSI'
    T_MPC = 'MPC'
    T_PREPROCESSING = 'PREPROCESSING'
    T_FEATURE_ENGINEERING = 'FEATURE_ENGINEERING'
    T_TRAINING = 'XGB_TRAINING'
    T_PREDICTION = 'XGB_PREDICTING'

    # finish job
    T_ON_SUCCESS = 'T_ON_SUCCESS'
    T_ON_FAILURE = 'T_ON_FAILURE'
