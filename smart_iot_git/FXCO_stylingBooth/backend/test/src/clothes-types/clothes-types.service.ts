import { Injectable } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { ClothesTypes } from 'src/entities/clothesTypes.entity';

@Injectable()
export class ClothesTypesService {
    constructor(
        @InjectRepository(ClothesTypes)
        private readonly clothesTypesRepository: Repository<ClothesTypes>,
    ) { }

    async create(data: Partial<ClothesTypes>): Promise<ClothesTypes> {
        const entity = this.clothesTypesRepository.create(data);
        return this.clothesTypesRepository.save(entity);
    }

    async findAll(): Promise<ClothesTypes[]> {
        return this.clothesTypesRepository.find();
    }

    async findOne(seq: number): Promise<ClothesTypes | null> {
        return this.clothesTypesRepository.findOne({ where: { seq } });
    }

    async update(seq: number, data: Partial<ClothesTypes>): Promise<ClothesTypes | null> {
        await this.clothesTypesRepository.update(seq, data);
        return this.findOne(seq);
    }

    async remove(seq: number): Promise<void> {
        await this.clothesTypesRepository.delete(seq);
    }
}
