import { Module } from '@nestjs/common';
import { JwtModule as NestJwtModule } from '@nestjs/jwt';

@Module({
  imports: [
    NestJwtModule.register({
        secret: 'f17a0fb7-7888-456f-8bae-98becce0ae47',
        signOptions: {
            expiresIn: '3650d',
        },
    }),
  ],
  exports: [NestJwtModule],
})
export class JwtModule {}
