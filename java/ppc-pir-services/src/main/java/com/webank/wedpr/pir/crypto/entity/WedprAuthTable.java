package com.webank.wedpr.pir.crypto.entity;

import java.time.LocalDateTime;
import javax.persistence.Entity;
import lombok.Data;

/**
 * @author zachma
 * @date 2024/9/3
 */
@Entity
@Data
public class WedprAuthTable {
    private String serviceId;
    private String accessKeyId;
    private LocalDateTime expireTime;
}
