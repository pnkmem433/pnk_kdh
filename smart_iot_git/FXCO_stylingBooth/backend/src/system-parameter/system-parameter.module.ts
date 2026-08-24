import { Module } from '@nestjs/common';
import { SystemParameterService } from './system-parameter.service';
import { TypeOrmModule } from '@nestjs/typeorm';
import { SystemParameter } from 'src/entities/systemParameter.entity';
import { SystemParameterController } from './system-parameter.controller';

@Module({
  imports: [TypeOrmModule.forFeature([SystemParameter])],
  providers: [SystemParameterService],
  controllers: [SystemParameterController]
})
export class SystemParameterModule {}
