import { Injectable, ConflictException, BadRequestException, UnauthorizedException, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { User } from '../../entites/user.entity';
import * as bcrypt from 'bcryptjs';
import { JwtService } from '@nestjs/jwt';
import { UserCreateDto } from 'src/dto/user/create.dto';
import { UserUpdateNameDto } from 'src/dto/user/update-name.dto';
import { UserUpdatePasswordDto } from 'src/dto/user/update-password';

@Injectable()
export class UsersService {
  constructor(
    @InjectRepository(User)
    private usersRepository: Repository<User>,
  ) { }

  async create(userRegisterDto: UserCreateDto): Promise<{ message: string }> {
    const existingUser = await this.usersRepository.findOne({ where: { id: userRegisterDto.id } });
    if (existingUser) {
      throw new BadRequestException('이미 사용 중인 아이디입니다. 다른 아이디를 선택해주세요.');
    }

    if (userRegisterDto.name.length < 3) {
      throw new BadRequestException('name은 최소 3글자 이상이어야 합니다.');
    }

    if (!/^[a-zA-Z0-9]{3,}$/.test(userRegisterDto.id)) {
      throw new BadRequestException('아이디는 3글자 이상이어야 하며, 영문 대소문자와 숫자만 포함할 수 있습니다.');
    }

    if (!/^[a-zA-Z0-9]{8,}$/.test(userRegisterDto.password)) {
      throw new BadRequestException('비밀번호는 8글자 이상이어야 하며, 영문 대소문자와 숫자만 포함할 수 있습니다.');
    }

    const hashedPassword = await bcrypt.hash(userRegisterDto.password, 10);

    const newUser = this.usersRepository.create({ id: userRegisterDto.id, name: userRegisterDto.name, password: hashedPassword });
    this.usersRepository.save(newUser);

    return { message: '사용자가 성공적으로 등록되었습니다' };
  }


  async updateName(user: User, updateNameDto: UserUpdateNameDto): Promise<{message: string, newName: string}> {
    user = await this.usersRepository.findOne({ where: { seq: user.seq } });

    if (updateNameDto.newName.length < 3) {
      throw new BadRequestException('name은 최소 3글자 이상이어야 합니다.');
    }

    if (!user) {
      throw new NotFoundException('사용자를 찾을 수 없습니다.');
    }
    user.name = updateNameDto.newName;
    await this.usersRepository.save(user);

    return { message: "사용자 이름이 성공적으로 변경되었습니다.", newName: '홍길동' };
  }

  async updatePassword(user: User, updatePasswordDto: UserUpdatePasswordDto): Promise<{message:string}> {
    user = await this.usersRepository.findOne({ where: { seq: user.seq } });

    if (!/^[a-zA-Z\d!@#$%^&*]{8,}$/.test(updatePasswordDto.new_password))
      throw new BadRequestException('새 비밀번호는 영문 대소문자, 숫자, 특수문자(!@#$%^&*)만 포함할 수 있으며, 최소 8글자 이상이어야 합니다.');

    if (!user)
      throw new NotFoundException('사용자를 찾을 수 없습니다.');

    if (!await bcrypt.compare(updatePasswordDto.current_password, user.password))
      throw new BadRequestException('현재 비밀번호가 올바르지 않습니다.');


    const hashedPassword = await bcrypt.hash(updatePasswordDto.new_password, 10);
    user.password = hashedPassword;
    await this.usersRepository.save(user);

    return { message: "사용자 비밀번호가 성공적으로 변경되었습니다."};
  }
}
