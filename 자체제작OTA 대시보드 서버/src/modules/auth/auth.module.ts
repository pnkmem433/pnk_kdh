import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';

import { JwtModule } from 'src/jwt/jwt.module';

import { AuthController } from './auth.controller';
import { AuthService } from './auth.service';

import { User } from '../../entites/user.entity';

@Module({
  imports: [JwtModule,TypeOrmModule.forFeature([User])],
  controllers: [AuthController],
  providers: [AuthService],
})
export class AuthModule { }
