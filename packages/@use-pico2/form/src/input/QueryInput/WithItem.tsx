import {
    type PicoSchema,
    type WithIdentitySchema
}                             from "@use-pico2/schema";
import {type ISelectionStore} from "@use-pico2/selection";
import {useStore}             from "@use-pico2/store";
import {
    Group,
    ModalStore
}                             from "@use-pico2/ui";
import {type FC}              from "react";
import type {ValuesSchema}    from "../../schema/ValuesSchema";
import {InputEx}              from "../InputEx";

export namespace WithItem {
    export interface Props<
        TValuesSchema extends ValuesSchema,
        TResponseSchema extends WithIdentitySchema,
    > extends InputEx.Props<TValuesSchema> {
        Item: Item<TResponseSchema>;
        SelectionStore: ISelectionStore<PicoSchema.Output<TResponseSchema>>;
    }

    export type Item<TResponseSchema extends WithIdentitySchema> = FC<ItemProps<TResponseSchema>>;

    export interface ItemProps<TResponseSchema extends WithIdentitySchema> {
        entity: PicoSchema.Output<TResponseSchema>;
    }
}

export const WithItem = <
    TValuesSchema extends ValuesSchema,
    TResponseSchema extends WithIdentitySchema,
>(
    {
        disabled,
        Item,
        SelectionStore,
        ...props
    }: WithItem.Props<TValuesSchema, TResponseSchema>
) => {
    const {open} = useStore(ModalStore, ({open}) => ({open}));
    const {
        clear,
        item
    } = useStore(SelectionStore, (
        {
            clear,
            item
        }) => ({
        clear,
        item
    }));
    return <InputEx
        disabled={disabled}
        onClick={disabled ? undefined : () => open("query-input")}
        onClear={disabled ? undefined : clear}
        {...props}
    >
        {item ? <Group
            gap={4}
            align={"center"}
        >
            <Item entity={item}/>
        </Group> : null}
    </InputEx>;
};
