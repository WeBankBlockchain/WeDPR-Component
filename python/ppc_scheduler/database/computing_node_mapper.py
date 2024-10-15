from sqlalchemy import update, and_, select, delete

from ppc_common.db_models.computing_node import ComputingNodeRecord


# def insert_computing_node(session, node_id: str, url: str, node_type: str, loading: int):
#     new_node = ComputingNodeRecord(
#         id=node_id,
#         url=url,
#         type=node_type,
#         loading=loading
#     )

#     session.add(new_node)


# def delete_computing_node(session, url: str, node_type: str):
#     stmt = (
#         delete(ComputingNodeRecord).where(
#             and_(
#                 ComputingNodeRecord.url == url,
#                 ComputingNodeRecord.type == node_type
#             )
#         )
#     )

#     result = session.execute(stmt)

#     return result.rowcount > 0


def get_and_update_min_loading_url(session, node_type):
    
    # select min_loading node
    min_loading_node_id = session.query(ComputingNodeRecord.id).filter(
        ComputingNodeRecord.type == node_type
    ).order_by(ComputingNodeRecord.loading.asc()).first()


    # update loading
    stmt = (
        update(ComputingNodeRecord).where(
            and_(
                ComputingNodeRecord.id == min_loading_node_id.id,
                ComputingNodeRecord.type == node_type
            )
        ).values(
            loading=ComputingNodeRecord.loading + 1
        )
    )
    session.execute(stmt)

    # select min_loading node
    record = session.query(ComputingNodeRecord.url, ComputingNodeRecord.token).filter(
        ComputingNodeRecord.id == min_loading_node_id.id
    ).order_by(ComputingNodeRecord.loading.asc()).first()
    
    return record

def release_loading(session, url: str, node_type: str):
    stmt = (
        update(ComputingNodeRecord).where(
            and_(
                ComputingNodeRecord.url == url,
                ComputingNodeRecord.type == node_type,
                ComputingNodeRecord.loading > 0
            )
        ).values(
            loading=ComputingNodeRecord.loading - 1
        )
    )
    result = session.execute(stmt)

    return result.rowcount > 0
