import {withService}         from "@use-pico/server";
import {type ItemRepository} from "../repository/ItemRepository";

export const withItemRepository = withService<ItemRepository.Type>("@derivean/item/ItemRepository");
export type withItemRepository = typeof withItemRepository["service"];
