import {IconSubtask} from "@tabler/icons-react";
import {tx}          from "@use-pico/i18n";
import {Page}        from "@use-pico/ui";

export default function Index() {
    return <Page
        icon={<IconSubtask/>}
        label={{
            title:  tx()`Game management`,
            header: tx()`Game management`,
        }}
    >
    </Page>;
}
