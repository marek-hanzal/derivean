import {withService}         from "@use-pico/container";
import {type ItemRepository} from "../repository/ItemRepository";

export const withItemRepository = withService<ItemRepository.Type>("@derivean/item/ItemRepository");
