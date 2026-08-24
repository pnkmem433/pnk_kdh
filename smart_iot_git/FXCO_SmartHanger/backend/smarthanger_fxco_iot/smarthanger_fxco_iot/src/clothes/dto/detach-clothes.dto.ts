import { ApiProperty } from '@nestjs/swagger';
import { IsString } from 'class-validator';

export class DetachClothesDto {
  @ApiProperty({
    example: 'hanger-uuid-001',
    description: '행거 UUID',
  })
  @IsString()
  hangerUuid: string;
}
