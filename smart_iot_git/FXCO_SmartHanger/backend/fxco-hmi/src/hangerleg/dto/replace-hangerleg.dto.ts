import { ApiProperty } from '@nestjs/swagger';
import { IsString, IsNotEmpty, IsInt, Min, IsOptional } from 'class-validator';

export class ReplaceHangerlegDto {
  @ApiProperty({ 
    description: '스마트 헹거랙 UUID', 
    example: '1D947649930000' 
  })
  @IsString()
  @IsNotEmpty()
  hangerlegUuid: string;

  @ApiProperty({ 
    description: '붙을 행거랙 시퀀스 번호', 
    example: 1,
    required: false
  })
  @IsInt()
  @Min(1)
  @IsOptional()
  rackSeq?: number;

  @ApiProperty({ 
    description: '순서 (1, 2, 3, 4, 5...)', 
    example: 1 
  })
  @IsInt()
  @Min(1)
  position: number;
}
