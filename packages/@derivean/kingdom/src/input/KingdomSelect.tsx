import {
    type ValuesSchema,
    withSourceQueryInput
}                              from "@use-pico/form";
import {
    type ComponentProps,
    useCallback
}                              from "react";
import {KingdomInline}         from "../inline/KingdomInline";
import {KingdomQueryStore}     from "../query/KingdomQueryStore";
import {KingdomRpc}            from "../rpc/KingdomRpc";
import {KingdomSelectionStore} from "../store/KingdomSelectionStore";
import {KingdomTable}          from "../table/KingdomTable";

const KingdomQueryInput = withSourceQueryInput({
    withQueryStore:  KingdomQueryStore,
    withSourceQuery: KingdomRpc.query,
    SelectionStore:  KingdomSelectionStore.single,
});
type KingdomQueryInput<
    TValuesSchema extends ValuesSchema,
> = typeof KingdomQueryInput<TValuesSchema>;

export namespace KingdomSelect {
    export interface Props<
        TValuesSchema extends ValuesSchema,
    > extends Omit<
        ComponentProps<KingdomQueryInput<TValuesSchema>>,
        "Selector" | "Item"
    > {
        tableProps?: Partial<ComponentProps<typeof KingdomTable>>;
    }
}

export const KingdomSelect = <
    TValuesSchema extends ValuesSchema,
>(
    {
        tableProps,
        ...props
    }: KingdomSelect.Props<TValuesSchema>
) => {
    return <KingdomQueryInput
        Selector={useCallback(() => <KingdomTable withLinkLock {...tableProps}/>, [])}
        Item={KingdomInline}
        {...props}
    />;
};
