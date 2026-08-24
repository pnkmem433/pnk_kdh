import { ApiProperty } from '@nestjs/swagger';
import { IsString, IsNotEmpty, IsInt, Min } from 'class-validator';

export class MatchClothesDto {
  @ApiProperty({ 
    description: '스마트 헹거 UUID', 
    example: '8E4EF94C-6A44-4F70-6E14-BC410EB0F021' 
  })
  @IsString()
  @IsNotEmpty()
  hangerUuid: string;

  @ApiProperty({ 
    description: '의류 시퀀스 번호', 
    example: 123 
  })
  @IsInt()
  @Min(1)
  clothesSeq: number;
}
