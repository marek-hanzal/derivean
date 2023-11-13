import {withService}             from "@use-pico/container";
import {type ItemTypeRepository} from "../repository/ItemTypeRepository";

export const withItemTypeRepository = withService<ItemTypeRepository.Type>("@derivean/item/ItemTypeRepository");
