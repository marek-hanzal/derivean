import {type WithIdentitySchema}       from "@use-pico2/schema";
import {type IStoreSchema}             from "@use-pico2/store";
import {type IBaseSelectionStoreProps} from "./IBaseSelectionStoreProps";

export type IBaseSelectionStore<
    TItem extends WithIdentitySchema.Type,
> = IStoreSchema<IBaseSelectionStoreProps<TItem>>["Store"];
