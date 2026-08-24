-- ============================================================
-- 스마트 헹거/헹거랙 교체 기능 구현을 위한 PostgreSQL CDC 마이그레이션
-- 작성일: 2026-01-27
-- 용도: PostgreSQL CDC 데이터베이스에 동일한 스키마 변경 적용
-- ============================================================

-- 1. rack 테이블 생성 (붙을 행거랙)
CREATE TABLE IF NOT EXISTS rack (
  seq SERIAL PRIMARY KEY,
  rack_number VARCHAR(50) NOT NULL UNIQUE,
  rack_location VARCHAR(255) NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- 2. hangerleg 테이블에 컬럼 추가
ALTER TABLE hangerleg 
ADD COLUMN IF NOT EXISTS rack_seq INTEGER NULL,
ADD COLUMN IF NOT EXISTS position INTEGER NULL;

-- 3. 외래키 제약조건 추가
DO $$
BEGIN
  IF NOT EXISTS (
    SELECT 1 FROM pg_constraint 
    WHERE conname = 'FK_hangerleg_rack'
  ) THEN
    ALTER TABLE hangerleg
    ADD CONSTRAINT FK_hangerleg_rack 
    FOREIGN KEY (rack_seq) REFERENCES rack(seq) 
    ON DELETE SET NULL ON UPDATE CASCADE;
  END IF;
END $$;

-- 4. 인덱스 추가 (성능 최적화)
CREATE INDEX IF NOT EXISTS idx_rack_position ON hangerleg (rack_seq, position);

-- ============================================================
-- 마이그레이션 완료 후 확인 쿼리
-- ============================================================

-- rack 테이블 구조 확인
-- \d rack;

-- hangerleg 테이블 구조 확인
-- \d hangerleg;

-- 외래키 제약조건 확인
-- SELECT 
--   tc.constraint_name, 
--   tc.table_name, 
--   kcu.column_name, 
--   ccu.table_name AS foreign_table_name,
--   ccu.column_name AS foreign_column_name 
-- FROM 
--   information_schema.table_constraints AS tc 
--   JOIN information_schema.key_column_usage AS kcu
--     ON tc.constraint_name = kcu.constraint_name
--   JOIN information_schema.constraint_column_usage AS ccu
--     ON ccu.constraint_name = tc.constraint_name
-- WHERE 
--   tc.constraint_type = 'FOREIGN KEY' 
--   AND tc.table_name = 'hangerleg'
--   AND tc.constraint_name = 'FK_hangerleg_rack';
