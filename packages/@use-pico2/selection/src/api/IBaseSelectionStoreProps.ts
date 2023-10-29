import {type WithIdentitySchema} from "@use-pico2/schema";
import {type IStoreProps}        from "@use-pico2/store";
import {type IBaseSelection}     from "./IBaseSelection";

export type IBaseSelectionStoreProps<
    TItem extends WithIdentitySchema.Type,
> = IStoreProps<IBaseSelection<TItem>>;
