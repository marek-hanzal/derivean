import {
    type ValuesSchema,
    withSourceQueryInput
}                           from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                           from "react";
import {ItemInline}         from "../inline/ItemInline";
import {ItemQueryStore}     from "../query/ItemQueryStore";
import {ItemRpc}            from "../rpc/ItemRpc";
import {ItemSelectionStore} from "../store/ItemSelectionStore";
import {ItemTable}          from "../table/ItemTable";

const ItemQueryInput = withSourceQueryInput({
    withQueryStore:  ItemQueryStore,
    withSourceQuery: ItemRpc.query,
    SelectionStore:  ItemSelectionStore.single,
});
type ItemQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof ItemQueryInput<TValuesSchema>;

export namespace ItemSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<ItemQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof ItemTable>>;
    }
}

export const ItemSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: ItemSelect.Props<TValuesSchema>
) => {
    return <ItemQueryInput
        Selector={useCallback(() => <ItemTable withLinkLock {...tableProps}/>, [])}
        Item={ItemInline}
        {...props}
    />;
};
