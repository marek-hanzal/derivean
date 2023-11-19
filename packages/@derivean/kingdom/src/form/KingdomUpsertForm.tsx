import {KingdomIcon}       from "@derivean/ui";
import {TextInput}         from "@use-pico/form";
import {
    t,
    tx
}                          from "@use-pico/i18n";
import {
    type ComponentProps,
    type FC
}                          from "react";
import {KingdomComponents} from "../ui/KingdomComponents";

export namespace KingdomUpsertForm {
    export type Props = Omit<ComponentProps<KingdomComponents["MutationForm"]>, "inputs" | "defaultValues" | "Render">;
}

export const KingdomUpsertForm: FC<KingdomUpsertForm.Props> = (
    {
        entity,
        ...props
    }
) => {
    return <KingdomComponents.MutationForm
        text={{
            submit:  entity ? t()`Update kingdom (label)` : t()`Create kingdom (label)`,
            success: {
                title:   t()`Success`,
                message: entity ? t()`Kingdom updated` : t()`Kingdom created`,
            }
        }}
        icon={<KingdomIcon/>}
        inputs={{
            name: props => <TextInput
                label={t()`Kingdom name`}
                placeholder={tx()`Kingdom name (placeholder)`}
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
