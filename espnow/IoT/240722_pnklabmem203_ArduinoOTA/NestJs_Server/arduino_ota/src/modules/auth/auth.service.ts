import { Injectable, UnauthorizedException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import * as bcrypt from 'bcryptjs';

import { User } from '../../entites/user.entity';
import { UserLoginDto } from '../../dto/user/login.dto';
import { JwtService } from '@nestjs/jwt';


@Injectable()
export class AuthService {
  constructor(
    @InjectRepository(User)
    private usersRepository: Repository<User>,
    private jwtService: JwtService,  // 생성자에 JwtService 주입
  ) { }


  async login(userLoginDto: UserLoginDto): Promise<{
    message: string;
    token: { access_token: string; refresh_token: string };
    profile: { name: string };
  }> {
    const user = await this.usersRepository.findOne({ where: { id: userLoginDto.id } });

    if (user && await bcrypt.compare(userLoginDto.password, user.password)) {
      const access_token = this.jwtService.sign(
        { seq: user.seq, type: 'access' },
        { expiresIn: '15m' },
      );

      const refresh_token = this.jwtService.sign(
        { seq: user.seq, type: 'refresh' },
        { expiresIn: '7d' },
      );

      // DB에 refresh token 저장 (hash 처리)
      const hashedRefreshToken = await bcrypt.hash(refresh_token, 10);
      await this.usersRepository.update(user.seq, { refreshToken: hashedRefreshToken });

      return {
        message: '로그인이 완료되었습니다.',
        token: { access_token, refresh_token },
        profile: { name: user.name },
      };
    } else {
      throw new UnauthorizedException('아이디 또는 비밀번호가 잘못되었습니다.');
    }
  }

  async refresh(refreshToken: string): Promise<{ message: string; token: { access_token: string; refresh_token: string } }> {
  try {
    const payload = this.jwtService.verify(refreshToken); // 예외 처리 필요
    if (payload.type !== 'refresh') {
      throw new UnauthorizedException('유효하지 않은 토큰 타입입니다.');
    }

    const user = await this.usersRepository.findOne({ where: { seq: payload.seq } });
    if (!user || !user.refreshToken) {
      throw new UnauthorizedException('사용자를 찾을 수 없습니다.');
    }

    const isTokenValid = await bcrypt.compare(refreshToken, user.refreshToken);
    if (!isTokenValid) {
      throw new UnauthorizedException('Refresh token이 일치하지 않습니다.');
    }

    // 새로운 access & refresh token 발급
    const newAccessToken = this.jwtService.sign({ seq: user.seq, type: 'access' }, { expiresIn: '15m' });
    const newRefreshToken = this.jwtService.sign({ seq: user.seq, type: 'refresh' }, { expiresIn: '7d' });

    const hashedRefreshToken = await bcrypt.hash(newRefreshToken, 10);
    await this.usersRepository.update(user.seq, { refreshToken: hashedRefreshToken });

    return {
      message: '토큰이 재발급되었습니다.',
      token: { access_token: newAccessToken, refresh_token: newRefreshToken },
    };
  } catch (err) {
    throw new UnauthorizedException('유효하지 않은 리프레시 토큰입니다.');
  }
}

}
