// dto/user/refresh-token.dto.ts
import { ApiProperty } from '@nestjs/swagger';

export class RefreshTokenDto {
  @ApiProperty({
    description: '클라이언트가 보유한 Refresh Token (JWT 형식)',
    example: 'jwt.refresh.token',
    type: 'string',
  })
  refresh_token: string;
}
