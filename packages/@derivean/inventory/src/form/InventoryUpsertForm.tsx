import {InventoryIcon} from "@derivean/ui";
import {TextInput}     from "@use-pico/form";
import {
    t,
    tx
}                      from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                      from "react";
import {InventoryUI}   from "../ui/InventoryUI";

export namespace InventoryUpsertForm {
    export type Props = Omit<ComponentProps<InventoryUI["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const InventoryUpsertForm: FC<InventoryUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <InventoryUI.MutationForm
        text={{
            submit:  entity ? t()`Update inventory item (label)` : t()`Create inventory item (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Inventory updated` : t()`Inventory created`,
            }
        }}
        icon={<InventoryIcon/>}
        inputs={{
            name: props => <TextInput
                label={t()`Inventory name`}
                placeholder={tx()`Inventory name (placeholder)`}
                {...props}
            />,
        }}
        values={entity}
        toRequest={values => (entity ? {
            update: {
                update: values,
                query:  {
                    where: {
                        id: entity.id,
                    },
                },
            },
        } : {
            create: values,
        })}
        defaultValues={{
            name: "",
        }}
        Render={({Input}) => <>
            <Input name={"name"}/>
        </>}
        {...props}
    />;
};
