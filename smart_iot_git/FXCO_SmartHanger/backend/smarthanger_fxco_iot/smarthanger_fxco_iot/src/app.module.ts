import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HangerModule } from './hanger/hanger.module';
import { ClothesModule } from './clothes/clothes.module';
import { HangerlegModule } from './hangerleg/hangerleg.module';
import { TimeTrackingModule } from './time-tracking/time-tracking.module';

@Module({
  imports: [
    TypeOrmModule.forRoot({
      type: 'mysql',
      host: 'host.docker.internal',
      port: 3306,
      username: 'root',
      password: '71805802',
      database: 'cc_nanaland_mvp',
      entities: [__dirname + '/**/*.entity{.ts,.js}'],
      synchronize: false,
    }),
    HangerlegModule,
    HangerModule,
    ClothesModule,
    TimeTrackingModule,
  ],
  controllers: [],
  providers: [],
})
export class AppModule { }
