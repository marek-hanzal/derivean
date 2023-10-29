import {type PicoSchema}      from "@use-pico2/schema";
import {type IStoreSchema}    from "@use-pico2/store/src/api/IStoreSchema";
import {type IListStoreProps} from "./IListStoreProps";

export type IListStore<TSchema extends PicoSchema> = IStoreSchema.Of<IListStoreProps<TSchema>>["Store"];
