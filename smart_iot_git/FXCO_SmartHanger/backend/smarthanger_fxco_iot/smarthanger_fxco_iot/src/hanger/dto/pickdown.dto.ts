import { ApiProperty } from '@nestjs/swagger';
import { IsString } from 'class-validator';

export class PickdownDto {
  @ApiProperty({
    example: 'hanger-uuid-001',
    description: '스마트 행거 UUID',
  })
  @IsString()
  uuid: string;

  @ApiProperty({
    example: 'rack-nfc-uuid-01',
    description: 'NFC(hangerleg) UUID',
  })
  @IsString()
  pickdownUuid: string;
}
