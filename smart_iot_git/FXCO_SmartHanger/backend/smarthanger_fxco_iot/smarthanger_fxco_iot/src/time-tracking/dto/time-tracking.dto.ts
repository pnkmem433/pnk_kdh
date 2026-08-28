import { ApiProperty, ApiPropertyOptional } from '@nestjs/swagger';
import { IsOptional, IsString } from 'class-validator';

export class TimeTrackingDto {
  @ApiProperty({ description: 'Hanger UUID', example: 'HANGER-ABC-123' })
  @IsString()
  uuid: string;

  @ApiProperty({ description: 'ms since device power-on', example: 3600000, type: 'integer' })
  startMs: number;

  @ApiProperty({ description: 'ms since device power-on', example: 5400000, type: 'integer' })
  endMs: number;

  @ApiPropertyOptional({ description: 'HangerLeg UUID (선택)', example: 'LEG-XYZ-999' })
  @IsOptional()
  @IsString()
  legUuid?: string;

  @ApiPropertyOptional({ description: "요청 시각. 'YYYY-MM-DD HH:mm:ss' 또는 epoch ms 문자열. null 또는 미전송 시 DB NOW(3) 사용" })
  @IsOptional()
  @IsString()
  requestTime?: string | null;

  // Backward-compat (deprecated)
  @ApiPropertyOptional({ description: 'DEPRECATED: start time YYYY-MM-DD HH:mm:ss', deprecated: true })
  @IsOptional()
  @IsString()
  startTime?: string;

  @ApiPropertyOptional({ description: 'DEPRECATED: end time YYYY-MM-DD HH:mm:ss', deprecated: true })
  @IsOptional()
  @IsString()
  endTime?: string;
}