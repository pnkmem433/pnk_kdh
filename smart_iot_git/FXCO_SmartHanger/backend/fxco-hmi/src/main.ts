import { NestFactory } from '@nestjs/core';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';
import { ValidationPipe } from '@nestjs/common';
import { AppModule } from './app.module';

async function bootstrap() {
  try {
    const app = await NestFactory.create(AppModule, {
      logger: ['error', 'warn', 'log'],
    });
    
    // CORS 설정 (필요시)
    app.enableCors();
    
    // 전역 ValidationPipe 설정
    app.useGlobalPipes(
      new ValidationPipe({
        whitelist: true,
        forbidNonWhitelisted: true,
        transform: true,
      }),
    );
    
    // 글로벌 prefix 설정 (필요시)
    // app.setGlobalPrefix('api');
    
    // Swagger 설정
    const config = new DocumentBuilder()
      .setTitle('FXCO HMI API')
      .setDescription('대구 FXCO 현장 스마트 행거 설치 HMI 백엔드 API 문서')
      .setVersion('1.0')
      .addTag('hanger', '행거 관련 API')
      .addTag('hangerleg', '행거랙 관련 API')
      .addTag('rack', '붙을 행거랙 관련 API')
      .addTag('hmi-content', 'HMI 콘텐츠 관련 API')
      .build();
    
    const document = SwaggerModule.createDocument(app, config);
    SwaggerModule.setup('api-docs', app, document);
    
    const port = process.env.PORT || 3000;
    await app.listen(port);
    
    console.log(`\n✅ FXCO HMI 서버가 포트 ${port}에서 실행 중입니다.`);
    console.log(`📚 Swagger API 문서: http://localhost:${port}/api-docs`);
  } catch (error) {
    console.error('\n❌ 서버 시작 실패:', error.message);
    if (error.message.includes('ECONNREFUSED') || error.message.includes('connect')) {
      console.error('\n데이터베이스 연결 오류가 발생했습니다.');
      console.error('다음 사항을 확인해주세요:');
      console.error('1. MySQL 서버가 실행 중인지 확인');
      console.error('2. .env 파일이 존재하고 올바른 설정이 있는지 확인');
      console.error('   - DB_HOST, DB_PORT, DB_USERNAME, DB_PASSWORD, DB_DATABASE');
      console.error('3. 데이터베이스가 생성되어 있는지 확인');
      console.error('4. 사용자 권한이 올바른지 확인');
      console.error('\n.env 파일 예시:');
      console.error('DB_HOST=localhost');
      console.error('DB_PORT=3306');
      console.error('DB_USERNAME=root');
      console.error('DB_PASSWORD=your_password');
      console.error('DB_DATABASE=fxco_db');
    }
    process.exit(1);
  }
}

bootstrap();

