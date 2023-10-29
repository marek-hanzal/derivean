import {type WithIdentitySchema} from "@use-pico2/schema";
import {type IStoreProps}        from "@use-pico2/store";
import {type IBaseSelection}     from "./IBaseSelection";

export type ISelectionStoreProps<
    TItem extends WithIdentitySchema.Type,
> = IStoreProps<
    {
        item?: TItem;
        selection?: TItem;
        required(): TItem;
    } & IBaseSelection<TItem>
>;
