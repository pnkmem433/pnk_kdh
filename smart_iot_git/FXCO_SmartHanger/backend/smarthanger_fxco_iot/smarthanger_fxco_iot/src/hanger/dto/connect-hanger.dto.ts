import { ApiProperty } from '@nestjs/swagger';
import { IsString } from 'class-validator';

export class ConnectHangerDto {
  @ApiProperty({ example: 'HANGER-UUID-1234' })
  @IsString()
  uuid: string;
}
