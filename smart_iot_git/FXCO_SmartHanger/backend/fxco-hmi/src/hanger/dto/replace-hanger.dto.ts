import { ApiProperty } from '@nestjs/swagger';
import { IsString, IsNotEmpty } from 'class-validator';

export class ReplaceHangerDto {
  @ApiProperty({ 
    description: '스마트 헹거 UUID', 
    example: '8E4EF94C-6A44-4F70-6E14-BC410EB0F021' 
  })
  @IsString()
  @IsNotEmpty()
  hangerUuid: string;

  @ApiProperty({ 
    description: '스마트 헹거랙 UUID', 
    example: '1D947649930000' 
  })
  @IsString()
  @IsNotEmpty()
  hangerlegUuid: string;
}
