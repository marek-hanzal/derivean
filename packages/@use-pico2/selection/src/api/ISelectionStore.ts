import {type WithIdentitySchema}   from "@use-pico2/schema";
import {type IStoreSchema}         from "@use-pico2/store";
import {type ISelectionStoreProps} from "./ISelectionStoreProps";

export type ISelectionStore<
    TItem extends WithIdentitySchema.Type,
> = IStoreSchema<ISelectionStoreProps<TItem>>["Store"];
