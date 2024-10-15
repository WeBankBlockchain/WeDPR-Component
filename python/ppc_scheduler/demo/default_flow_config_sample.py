flow_dict = {

    "SHELL": [
        {
            "index": 1,
            "type": "T_SHELL"
        },
        {
            "index": 2,
            "type": "T_SHELL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        },
        {
            "index": 3,
            "type": "T_SHELL",
            "upstreams": [
                {
                    "index": 2
                }
            ]
        }
    ],

    "PSI": [
        {
            "index": 1,
            "type": "T_PSI"
        }
    ],

    "MPC": [
        {
            "index": 1,
            "type": "T_MPC"
        }
    ],

    "PSI_MPC": [
        {
            "index": 1,
            "type": "T_PSI"
        },
        {
            "index": 2,
            "type": "T_MPC",
            "upstreams": [
                {
                    "index": 1,
                    "output_input_map": [
                        "0:0"
                    ]
                }
            ]
        }
    ],

    "PREPROCESSING": [
        {
            "index": 1,
            "type": "T_MODEL"
        }
    ],

    "FEATURE_ENGINEERING": [
        {
            "index": 1,
            "type": "T_MODEL"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        }
    ],

    "TRAINING": [
        {
            "index": 1,
            "type": "T_MODEL"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        }
    ],

    "PREDICTION": [
        {
            "index": 1,
            "type": "T_MODEL"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        }
    ],

    "FEATURE_ENGINEERING_TRAINING": [
        {
            "index": 1,
            "type": "T_MODEL"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        },
        {
            "index": 3,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 2
                }
            ]
        }
    ],

    "PSI_FEATURE_ENGINEERING": [
        {
            "index": 1,
            "type": "T_PSI"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        },
        {
            "index": 3,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 2
                }
            ]
        }
    ],

    "PSI_TRAINING": [
        {
            "index": 1,
            "type": "T_PSI"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        },
        {
            "index": 3,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 2
                }
            ]
        }
    ],

    "PSI_FEATURE_ENGINEERING_TRAINING": [
        {
            "index": 1,
            "type": "T_PSI"
        },
        {
            "index": 2,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 1
                }
            ]
        },
        {
            "index": 3,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 2
                }
            ]
        },
        {
            "index": 4,
            "type": "T_MODEL",
            "upstreams": [
                {
                    "index": 3
                }
            ]
        }
    ]
}
