from enum import Enum

from ppc_common.ppc_utils.exception import PpcException, PpcErrorCode
from ppc_common.ppc_utils.utils import JobRole, JobStatus


def check_job_status(job_status):
    if job_status in JobStatus.__members__:
        return True
    return False


def check_job_role(job_role):
    if job_role in JobRole.__members__:
        return True
    return False


ADMIN_PERMISSIONS = 'ADMIN_PERMISSIONS'


class UserRole(Enum):
    ADMIN = 1
    DATA_PROVIDER = 2
    ALGO_PROVIDER = 3
    DATA_CONSUMER = 4


class PermissionGroup(Enum):
    AGENCY_GROUP = 1
    DATA_GROUP = 2
    ALGO_GROUP = 3
    JOB_GROUP = 4
    AUDIT_GROUP = 5


class AgencyGroup(Enum):
    LIST_AGENCY = 1
    WRITE_AGENCY = 2


class DataGroup(Enum):
    LIST_DATA = 1
    READ_DATA_PUBLIC_INFO = 2
    READ_DATA_PRIVATE_INFO = 3
    WRITE_DATA = 4


class AlgoGroup(Enum):
    LIST_ALGO = 1
    READ_ALGO_PUBLIC_INFO = 2
    READ_ALGO_PRIVATE_INFO = 3
    WRITE_ALGO = 4


class JobGroup(Enum):
    LIST_JOB = 1
    READ_JOB_PUBLIC_INFO = 2
    READ_JOB_PRIVATE_INFO = 3
    WRITE_JOB = 4


class AuditGroup(Enum):
    READ_AUDIT = 1
    WRITE_AUDIT = 2


# permissions formed as permission_a|permission_b|permission_a_group|...
def check_permission(permissions, needed_permission_group, *needed_permissions):
    permission_list = permissions.split('|')
    if ADMIN_PERMISSIONS in permission_list:
        return 0
    if needed_permission_group in permission_list:
        return 1
    for needed_permission in needed_permissions:
        if needed_permission in permission_list:
            return 1
    raise PpcException(PpcErrorCode.INSUFFICIENT_AUTHORITY.get_code(
    ), PpcErrorCode.INSUFFICIENT_AUTHORITY.get_msg())
