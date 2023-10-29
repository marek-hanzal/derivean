import {type WithIdentitySchema}        from "@use-pico2/schema";
import {type IStoreSchema}              from "@use-pico2/store";
import {type IMultiSelectionStoreProps} from "./IMultiSelectionStoreProps";

export type IMultiSelectionStore<
    TItem extends WithIdentitySchema.Type,
> = IStoreSchema<IMultiSelectionStoreProps<TItem>>["Store"];
