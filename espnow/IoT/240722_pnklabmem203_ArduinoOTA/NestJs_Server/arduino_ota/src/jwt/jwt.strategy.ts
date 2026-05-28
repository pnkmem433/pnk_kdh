import { Injectable } from '@nestjs/common';
import { PassportStrategy } from '@nestjs/passport';
import { verify } from 'crypto';
import { Strategy, ExtractJwt } from 'passport-jwt';

@Injectable()
export class JwtStrategy extends PassportStrategy(Strategy) {
  constructor() {
    super({
      jwtFromRequest: ExtractJwt.fromAuthHeaderAsBearerToken(),
      ignoreExpiration: false,
      secretOrKey: 'f17a0fb7-7888-456f-8bae-98becce0ae47',
    });
  }

  async validate(payload: any) {
    return { seq: payload.seq, type: payload.type };
  }
}
