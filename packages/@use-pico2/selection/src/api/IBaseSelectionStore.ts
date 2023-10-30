import {type WithIdentitySchema} from "@use-pico2/schema";
import {type IStore}             from "@use-pico2/store";
import {type IBaseSelection}     from "./IBaseSelection";

export type IBaseSelectionStore<
    TItem extends WithIdentitySchema.Type,
> = IStore.Store<IBaseSelectionStore.Store<TItem>>;

export namespace IBaseSelectionStore {
    export type Store<
        TItem extends WithIdentitySchema.Type,
    > = IStore<IBaseSelection<TItem>>;
}
