import { ApiProperty } from '@nestjs/swagger';
import { IsString } from 'class-validator';

export class AttachClothesDto {
  @ApiProperty({
    example: 'hanger-uuid-001',
    description: '행거 UUID',
  })
  @IsString()
  hangerUuid: string;

  @ApiProperty({
    example: 'CN11687',
    description: '옷 TAG',
  })
  @IsString()
  clothesTag: string;
}
