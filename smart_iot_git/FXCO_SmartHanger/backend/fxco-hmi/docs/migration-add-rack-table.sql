-- ============================================================
-- 스마트 헹거/헹거랙 교체 기능 구현을 위한 데이터베이스 마이그레이션
-- 작성일: 2026-01-27
-- ============================================================

-- 1. rack 테이블 생성 (붙을 행거랙)
CREATE TABLE IF NOT EXISTS `rack` (
  `seq` int NOT NULL AUTO_INCREMENT,
  `rack_number` VARCHAR(50) NOT NULL,
  `rack_location` VARCHAR(255) NULL,
  `created_at` DATETIME DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`seq`),
  UNIQUE KEY `rack_number` (`rack_number`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- 2. hangerleg 테이블에 컬럼 추가
ALTER TABLE `hangerleg` 
ADD COLUMN `rack_seq` INT NULL AFTER `uuid`,
ADD COLUMN `position` INT NULL AFTER `rack_seq`;

-- 3. 외래키 제약조건 추가
ALTER TABLE `hangerleg`
ADD CONSTRAINT `FK_hangerleg_rack` FOREIGN KEY (`rack_seq`) REFERENCES `rack` (`seq`) ON DELETE SET NULL ON UPDATE CASCADE;

-- 4. 인덱스 추가 (성능 최적화)
CREATE INDEX `idx_rack_position` ON `hangerleg` (`rack_seq`, `position`);

-- ============================================================
-- 마이그레이션 완료 후 확인 쿼리
-- ============================================================

-- rack 테이블 구조 확인
-- DESCRIBE `rack`;

-- hangerleg 테이블 구조 확인
-- DESCRIBE `hangerleg`;

-- 외래키 제약조건 확인
-- SELECT 
--   CONSTRAINT_NAME, 
--   TABLE_NAME, 
--   COLUMN_NAME, 
--   REFERENCED_TABLE_NAME, 
--   REFERENCED_COLUMN_NAME 
-- FROM 
--   INFORMATION_SCHEMA.KEY_COLUMN_USAGE 
-- WHERE 
--   TABLE_SCHEMA = DATABASE() 
--   AND TABLE_NAME = 'hangerleg' 
--   AND CONSTRAINT_NAME = 'FK_hangerleg_rack';
