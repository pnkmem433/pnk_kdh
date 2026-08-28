import { Module } from '@nestjs/common';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { TypeOrmModule } from '@nestjs/typeorm';
import { join } from 'path';
import { AppController } from './app.controller';
import { HangerModule } from './hanger/hanger.module';
import { ClothesModule } from './clothes/clothes.module';
import { HangerlegModule } from './hangerleg/hangerleg.module';
import { HmiContentModule } from './hmi-content/hmi-content.module';
import { RackModule } from './rack/rack.module';
import { Hanger } from './entities/hanger.entity';
import { Clothes } from './entities/clothes.entity';
import { HangerLog } from './entities/hanger-log.entity';
import { Hangerleg } from './entities/hangerleg.entity';
import { HmiContent } from './entities/hmi-content.entity';
import { Rack } from './entities/rack.entity';

@Module({
  imports: [
    ConfigModule.forRoot({
      isGlobal: true,
      envFilePath: [
        join(__dirname, '..', '.env'), // 프로젝트 루트의 .env 파일
        '.env', // 현재 디렉토리
      ],
      ignoreEnvFile: false,
      expandVariables: true,
    }),
    TypeOrmModule.forRootAsync({
      imports: [ConfigModule],
      useFactory: (configService: ConfigService) => {
        const dbConfig = {
          type: 'mysql' as const,
          host: configService.get<string>('DB_HOST', 'localhost'),
          port: parseInt(configService.get<string>('DB_PORT', '3306'), 10),
          username: configService.get<string>('DB_USERNAME', 'root'),
          password: configService.get<string>('DB_PASSWORD', ''),
          database: configService.get<string>('DB_DATABASE', 'fxco_db'),
          entities: [Hanger, Clothes, HangerLog, Hangerleg, HmiContent, Rack],
          synchronize: false, // 기존 테이블 사용 - 자동 생성/수정 비활성화
          logging: configService.get<string>('NODE_ENV') === 'development',
          retryAttempts: 3,
          retryDelay: 3000,
          autoLoadEntities: true,
        };

        // 개발 환경에서 연결 정보 로깅 (비밀번호 제외)
        console.log('\n=== 데이터베이스 연결 설정 ===');
        console.log(`현재 작업 디렉토리: ${process.cwd()}`);
        console.log(`환경 변수에서 읽은 값:`);
        console.log(`  DB_HOST: ${configService.get<string>('DB_HOST')} (기본값: localhost)`);
        console.log(`  DB_PORT: ${configService.get<string>('DB_PORT')} (기본값: 3306)`);
        console.log(`  DB_USERNAME: ${configService.get<string>('DB_USERNAME')} (기본값: root)`);
        console.log(`  DB_DATABASE: ${configService.get<string>('DB_DATABASE')} (기본값: fxco_db)`);
        console.log(`실제 사용할 설정:`);
        console.log(`  Host: ${dbConfig.host}`);
        console.log(`  Port: ${dbConfig.port}`);
        console.log(`  Username: ${dbConfig.username}`);
        console.log(`  Database: ${dbConfig.database}`);
        console.log('============================\n');

        return dbConfig;
      },
      inject: [ConfigService],
    }),
    HangerModule,
    ClothesModule,
    HangerlegModule,
    HmiContentModule,
    RackModule,
  ],
  controllers: [AppController],
  providers: [],
})
export class AppModule {}

