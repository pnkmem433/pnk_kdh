import { ApiProperty } from '@nestjs/swagger';
import { IsString } from 'class-validator';

export class PickupDto {
  @ApiProperty({
    example: 'hanger-uuid-001',
    description: '스마트 행거 UUID',
  })
  @IsString()
  uuid: string;
}
